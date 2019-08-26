#include "rtp_prompt.h"

Common::rtp_packet_t * m_pPromptdata;
unsigned long m_maxPacketIndex;
unsigned int m_ssrc;
std::string m_sFilenamepath;
		rtp_prompt::rtp_prompt()
		{
			m_pPromptdata = NULL;
			m_maxPacketIndex = 0;
			m_ssrc = rand();
			_logger = GlobalLogger::StaticLogger;
		}


		rtp_prompt::~rtp_prompt()
		{
			unload();
		}
	
		bool rtp_prompt::load(const std::string promptfile, unsigned int payloadType)
		{
			try{
				FILE * pfile = fopen(promptfile.c_str(), "rb");
				if (!pfile)
					return false;
	
				fseek(pfile, 0L, SEEK_END);
				long filesize = ftell(pfile);
				if (filesize < MIN_PROMPT_FILE_SIZE)
				{
					fclose(pfile);
					return false;
				}
	
				fseek(pfile, 0L, SEEK_SET);
				m_maxPacketIndex = filesize  / 160;
				packetCount = m_maxPacketIndex;
				m_pPromptdata = new Common::rtp_packet_t[m_maxPacketIndex];
				if (!m_pPromptdata)
				{
					fclose(pfile);
					return false;
				}
	
				unsigned short seq = 0;
				for (unsigned int Index = 0; Index < m_maxPacketIndex; Index++, seq++)
				{
					
					size_t readdata = fread(m_pPromptdata[Index].data, 2, 80, pfile);
					if (readdata != 80)
					{
						fclose(pfile);
						delete[]m_pPromptdata;
						m_pPromptdata = NULL;
						m_maxPacketIndex = 0;
						return false;
					}
					m_pPromptdata[Index].hdr.version = 2;
					m_pPromptdata[Index].hdr.p = 0;
					m_pPromptdata[Index].hdr.x = 0;
					m_pPromptdata[Index].hdr.m = 0;
					m_pPromptdata[Index].hdr.pt = payloadType;
					m_pPromptdata[Index].hdr.seq = htons(seq);
					m_pPromptdata[Index].hdr.ts = htonl(Index * (unsigned int)sizeof(m_pPromptdata[Index].data)); 
					m_pPromptdata[Index].hdr.ssrc = m_ssrc;
				}//end of for
	
				m_sFilenamepath = promptfile;
				fclose(pfile);
				return true;
	
			}
			catch (...)
			{
				std::exception_ptr ex = std::current_exception();
				_logger->error("rtp_prompt::load","Error in loading sound file "+ promptfile + string(ex.__cxa_exception_type()->name()));
				return false;
			}
		}
		
	
		bool rtp_prompt::unload()
		{
			m_sFilenamepath = "";
			m_maxPacketIndex = 0;
			if (m_pPromptdata)
			{
				delete[]m_pPromptdata;
				m_pPromptdata = NULL;
				return true;
			}
			else
				return false;
		}
	
		Common::rtp_packet_t * rtp_prompt::getPacket(unsigned long & Index)
		{
			Common::rtp_packet_t * pret = &(m_pPromptdata[Index]);
			Index = (Index + 1) % m_maxPacketIndex;
			return pret;

			/*if (m_pPromptdata && Index < (m_maxPacketIndex))
			{
				Common::rtp_packet_t * pret = &(m_pPromptdata[Index]);
				Index = (Index + 1) % m_maxPacketIndex;
				return pret;
			}
			else
				return NULL;*/
		}
	
